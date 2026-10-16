/**
 * 现代化主题 JavaScript - 修复版本
 * 解决菜单切换冲突问题
 */

$(document).ready(function() {
    
    // 防止重复绑定事件
    $('#menu_toggle').off('click.modern-theme');
    $('.side-menu > li > a').off('click.modern-theme');
    
    // 侧边栏切换功能
    $('#menu_toggle').on('click.modern-theme', function(e) {
        e.preventDefault();
        e.stopPropagation();
        
        console.log('Menu toggle clicked');
        
        // 切换nav-sm类
        $('body').toggleClass('nav-sm');
        
        // 移动端特殊处理
        if ($(window).width() <= 768) {
            $('.left_col').toggleClass('show');
        }
        
        // 添加切换动画
        $('.left_col').css('transition', 'all 0.3s ease');
        $('.right_col').css('transition', 'all 0.3s ease');
        
        // 延迟后重新计算布局
        setTimeout(function() {
            $(window).trigger('resize');
        }, 300);
    });
    
    // 子菜单切换功能
    $('.side-menu > li > a').on('click.modern-theme', function(e) {
        var $this = $(this);
        var $parent = $this.parent();
        var $submenu = $this.next('.child_menu');
        
        // 如果有子菜单，阻止默认行为
        if ($submenu.length > 0) {
            e.preventDefault();
            e.stopPropagation();
            
            console.log('Submenu toggle clicked');
            
            // 关闭其他子菜单
            $('.side-menu > li').not($parent).removeClass('active');
            $('.child_menu').not($submenu).slideUp(200);
            
            // 切换当前子菜单
            $parent.toggleClass('active');
            $submenu.slideToggle(200);
            
            // 旋转箭头图标
            var $icon = $this.find('.fa-chevron-down, .fa-chevron-up');
            if ($parent.hasClass('active')) {
                $icon.removeClass('fa-chevron-down').addClass('fa-chevron-up');
            } else {
                $icon.removeClass('fa-chevron-up').addClass('fa-chevron-down');
            }
        }
    });
    
    // 移动端点击遮罩关闭侧边栏
    $(document).on('click.modern-theme', function(e) {
        if ($(window).width() <= 768) {
            var $target = $(e.target);
            if (!$target.closest('.left_col').length && !$target.closest('#menu_toggle').length) {
                $('.left_col').removeClass('show');
            }
        }
    });
    
    // 窗口大小改变时的处理
    $(window).on('resize.modern-theme', function() {
        var width = $(window).width();
        
        if (width > 768) {
            $('.left_col').removeClass('show');
            if ($('body').hasClass('nav-sm')) {
                // 小侧边栏模式
                $('.right_col').css('margin-left', '70px').css('width', 'calc(100% - 70px)');
                $('.top_nav').css('width', 'calc(100% - 70px)');
            } else {
                // 正常侧边栏模式
                $('.right_col').css('margin-left', '230px').css('width', 'calc(100% - 230px)');
                $('.top_nav').css('width', 'calc(100% - 230px)');
            }
        } else {
            // 移动端模式
            $('.right_col').css('margin-left', '0').css('width', '100%');
            $('.top_nav').css('width', '100%');
        }
    });
    
    // 初始化时触发一次resize
    $(window).trigger('resize');
    
    // 卡片悬停效果
    $('.x_panel').hover(
        function() {
            $(this).addClass('hovered');
        },
        function() {
            $(this).removeClass('hovered');
        }
    );
    
    // 按钮点击波纹效果
    $('.btn').on('click', function(e) {
        addRippleEffect($(this), e);
    });
    
    // 平滑滚动
    $('a[href^="#"]').on('click', function(e) {
        e.preventDefault();
        var target = $(this.getAttribute('href'));
        if (target.length) {
            $('html, body').animate({
                scrollTop: target.offset().top - 100
            }, 800);
        }
    });
    
    // 工具提示初始化
    $('[data-toggle="tooltip"]').tooltip({
        container: 'body',
        animation: true,
        delay: { show: 500, hide: 100 }
    });
    
    // 进度条动画
    animateProgressBars();
    
    // 数字计数动画
    animateCounters();
    
    // 实时时间更新
    updateSystemTime();
    setInterval(updateSystemTime, 1000);
    
    // 通知系统
    initNotificationSystem();
    
    // 键盘快捷键
    initKeyboardShortcuts();
    
    console.log('🚀 现代化主题已加载 - 修复版本');
});

/**
 * 添加波纹效果
 */
function addRippleEffect($element, event) {
    var $ripple = $('<span class="ripple"></span>');
    var rect = $element[0].getBoundingClientRect();
    var size = Math.max(rect.width, rect.height);
    var x = event.clientX - rect.left - size / 2;
    var y = event.clientY - rect.top - size / 2;
    
    $ripple.css({
        position: 'absolute',
        width: size + 'px',
        height: size + 'px',
        left: x + 'px',
        top: y + 'px',
        background: 'rgba(255, 255, 255, 0.3)',
        borderRadius: '50%',
        transform: 'scale(0)',
        animation: 'ripple-animation 0.6s ease-out',
        pointerEvents: 'none',
        zIndex: 1000
    });
    
    $element.css('position', 'relative').append($ripple);
    
    setTimeout(function() { $ripple.remove(); }, 600);
}

/**
 * 进度条动画
 */
function animateProgressBars() {
    $('.progress-bar').each(function() {
        var $bar = $(this);
        var width = $bar.data('width') || $bar.css('width');
        
        $bar.css('width', '0%');
        
        setTimeout(function() {
            $bar.animate({ width: width }, 1500);
        }, 500);
    });
}

/**
 * 数字计数动画
 */
function animateCounters() {
    $('.stats-number').each(function() {
        var $counter = $(this);
        var target = parseInt($counter.text()) || 0;
        
        $counter.text('0');
        
        $({ count: 0 }).animate({ count: target }, {
            duration: 2000,
            step: function() {
                $counter.text(Math.floor(this.count));
            },
            complete: function() {
                $counter.text(target);
            }
        });
    });
}

/**
 * 更新系统时间
 */
function updateSystemTime() {
    var now = new Date();
    var timeString = now.toLocaleTimeString('zh-CN', { 
        hour12: false,
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
    
    $('#current_time').text(timeString);
}

/**
 * 初始化通知系统
 */
function initNotificationSystem() {
    // 模拟实时通知
    var notifications = [
        { type: 'info', message: '系统运行正常', icon: 'fa-info-circle' },
        { type: 'warning', message: '检测到异常行为', icon: 'fa-exclamation-triangle' },
        { type: 'success', message: '算法更新完成', icon: 'fa-check-circle' }
    ];
    
    setInterval(function() {
        if (Math.random() > 0.9) { // 10% 概率显示通知
            var notification = notifications[Math.floor(Math.random() * notifications.length)];
            showNotification(notification);
        }
    }, 10000);
}

/**
 * 显示通知
 */
function showNotification(notification) {
    var $notification = $('<div class="modern-notification ' + notification.type + '">' +
        '<i class="fa ' + notification.icon + '"></i>' +
        '<span>' + notification.message + '</span>' +
        '<button class="close-btn">&times;</button>' +
        '</div>');
    
    $('body').append($notification);
    
    // 显示动画
    setTimeout(function() { $notification.addClass('show'); }, 100);
    
    // 自动隐藏
    setTimeout(function() {
        $notification.removeClass('show');
        setTimeout(function() { $notification.remove(); }, 300);
    }, 5000);
    
    // 手动关闭
    $notification.find('.close-btn').on('click', function() {
        $notification.removeClass('show');
        setTimeout(function() { $notification.remove(); }, 300);
    });
}

/**
 * 初始化键盘快捷键
 */
function initKeyboardShortcuts() {
    $(document).on('keydown.modern-theme', function(e) {
        // Ctrl + / 显示快捷键帮助
        if (e.ctrlKey && e.key === '/') {
            e.preventDefault();
            showShortcutHelp();
        }
        
        // Ctrl + M 切换侧边栏
        if (e.ctrlKey && e.key === 'm') {
            e.preventDefault();
            $('#menu_toggle').trigger('click');
        }
        
        // ESC 关闭模态框
        if (e.key === 'Escape') {
            $('.modal').modal('hide');
            $('.modern-notification').removeClass('show');
        }
    });
}

/**
 * 显示快捷键帮助
 */
function showShortcutHelp() {
    var shortcuts = [
        { key: 'Ctrl + M', desc: '切换侧边栏' },
        { key: 'Ctrl + /', desc: '显示快捷键帮助' },
        { key: 'ESC', desc: '关闭弹窗' }
    ];
    
    var helpContent = '<div class="shortcut-help"><h4>键盘快捷键</h4><ul>';
    shortcuts.forEach(function(shortcut) {
        helpContent += '<li><kbd>' + shortcut.key + '</kbd> - ' + shortcut.desc + '</li>';
    });
    helpContent += '</ul></div>';
    
    showNotification({
        type: 'info',
        message: helpContent,
        icon: 'fa-keyboard-o'
    });
}

// 添加 CSS 动画样式
var animationStyles = 
'<style>' +
'@keyframes ripple-animation {' +
'    to {' +
'        transform: scale(4);' +
'        opacity: 0;' +
'    }' +
'}' +
'' +
'.modern-notification {' +
'    position: fixed;' +
'    top: 20px;' +
'    right: 20px;' +
'    background: var(--card-bg);' +
'    border: 1px solid var(--border-color);' +
'    border-radius: var(--border-radius);' +
'    padding: 15px 20px;' +
'    box-shadow: var(--shadow-xl);' +
'    transform: translateX(100%);' +
'    transition: all 0.3s ease;' +
'    z-index: 10000;' +
'    max-width: 300px;' +
'    backdrop-filter: blur(10px);' +
'}' +
'' +
'.modern-notification.show {' +
'    transform: translateX(0);' +
'}' +
'' +
'.modern-notification.info { border-left: 4px solid #4facfe; }' +
'.modern-notification.warning { border-left: 4px solid #fee140; }' +
'.modern-notification.success { border-left: 4px solid #43e97b; }' +
'' +
'.modern-notification .close-btn {' +
'    background: none;' +
'    border: none;' +
'    color: var(--text-muted);' +
'    float: right;' +
'    font-size: 18px;' +
'    cursor: pointer;' +
'    margin-left: 10px;' +
'}' +
'' +
'.shortcut-help {' +
'    color: var(--text-primary);' +
'}' +
'' +
'.shortcut-help kbd {' +
'    background: var(--hover-bg);' +
'    color: var(--text-primary);' +
'    border: 1px solid var(--border-color);' +
'    border-radius: 4px;' +
'    padding: 2px 6px;' +
'    font-size: 12px;' +
'}' +
'' +
'@media (max-width: 768px) {' +
'    .modern-notification {' +
'        right: 10px;' +
'        left: 10px;' +
'        max-width: none;' +
'    }' +
'}' +
'</style>';

$('head').append(animationStyles);