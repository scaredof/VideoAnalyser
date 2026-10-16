/**
 * 现代化主题 JavaScript
 * 提供动画效果、交互功能和用户体验增强
 */

$(document).ready(function() {
    
    // 初始化主题
    initModernTheme();
    
    // 侧边栏切换动画
    $('#menu_toggle').on('click', function() {
        $('body').toggleClass('nav-sm');
        $('.left_col').toggleClass('collapsed');
        
        // 添加切换动画
        $('.left_col').css('transition', 'all 0.3s ease');
        $('.right_col').css('transition', 'all 0.3s ease');
    });
    
    // 菜单项点击效果
    $('.side-menu li a').on('click', function(e) {
        const $this = $(this);
        const $parent = $this.parent();
        
        // 如果有子菜单
        if ($this.next('.child_menu').length > 0) {
            e.preventDefault();
            
            // 切换子菜单
            $parent.toggleClass('active');
            $this.next('.child_menu').slideToggle(300);
            
            // 旋转箭头图标
            $this.find('.fa-chevron-down, .fa-chevron-up').toggleClass('fa-chevron-down fa-chevron-up');
        }
        
        // 添加点击波纹效果
        addRippleEffect($this, e);
    });
    
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
        const target = $(this.getAttribute('href'));
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
    
    // 响应式处理
    handleResponsive();
    $(window).resize(handleResponsive);
    
    // 键盘快捷键
    initKeyboardShortcuts();
});

/**
 * 初始化现代化主题
 */
function initModernTheme() {
    // 添加主题类
    $('body').addClass('modern-theme');
    
    // 设置主题色彩变量
    const root = document.documentElement;
    root.style.setProperty('--theme-transition', 'all 0.3s cubic-bezier(0.4, 0, 0.2, 1)');
    
    // 预加载关键资源
    preloadResources();
    
    // 初始化动画观察器
    initAnimationObserver();
    
    console.log('🚀 现代化主题已加载');
}

/**
 * 添加波纹效果
 */
function addRippleEffect($element, event) {
    const $ripple = $('<span class="ripple"></span>');
    const rect = $element[0].getBoundingClientRect();
    const size = Math.max(rect.width, rect.height);
    const x = event.clientX - rect.left - size / 2;
    const y = event.clientY - rect.top - size / 2;
    
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
    
    setTimeout(() => $ripple.remove(), 600);
}

/**
 * 进度条动画
 */
function animateProgressBars() {
    $('.progress-bar').each(function() {
        const $bar = $(this);
        const width = $bar.data('width') || $bar.css('width');
        
        $bar.css('width', '0%');
        
        setTimeout(() => {
            $bar.animate({ width: width }, 1500, 'easeOutCubic');
        }, 500);
    });
}

/**
 * 数字计数动画
 */
function animateCounters() {
    $('.stats-number').each(function() {
        const $counter = $(this);
        const target = parseInt($counter.text()) || 0;
        
        $counter.text('0');
        
        $({ count: 0 }).animate({ count: target }, {
            duration: 2000,
            easing: 'easeOutCubic',
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
    const now = new Date();
    const timeString = now.toLocaleTimeString('zh-CN', { 
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
    const notifications = [
        { type: 'info', message: '系统运行正常', icon: 'fa-info-circle' },
        { type: 'warning', message: '检测到异常行为', icon: 'fa-exclamation-triangle' },
        { type: 'success', message: '算法更新完成', icon: 'fa-check-circle' }
    ];
    
    setInterval(() => {
        if (Math.random() > 0.7) { // 30% 概率显示通知
            const notification = notifications[Math.floor(Math.random() * notifications.length)];
            showNotification(notification);
        }
    }, 10000);
}

/**
 * 显示通知
 */
function showNotification(notification) {
    const $notification = $(`
        <div class="modern-notification ${notification.type}">
            <i class="fa ${notification.icon}"></i>
            <span>${notification.message}</span>
            <button class="close-btn">&times;</button>
        </div>
    `);
    
    $('body').append($notification);
    
    // 显示动画
    setTimeout(() => $notification.addClass('show'), 100);
    
    // 自动隐藏
    setTimeout(() => {
        $notification.removeClass('show');
        setTimeout(() => $notification.remove(), 300);
    }, 5000);
    
    // 手动关闭
    $notification.find('.close-btn').on('click', function() {
        $notification.removeClass('show');
        setTimeout(() => $notification.remove(), 300);
    });
}

/**
 * 响应式处理
 */
function handleResponsive() {
    const width = $(window).width();
    
    if (width <= 768) {
        $('body').addClass('mobile-view');
        $('.left_col').addClass('mobile-hidden');
    } else {
        $('body').removeClass('mobile-view');
        $('.left_col').removeClass('mobile-hidden');
    }
    
    // 调整图表大小
    if (typeof echarts !== 'undefined') {
        echarts.getInstanceByDom(document.getElementById('cpuCharts'))?.resize();
        echarts.getInstanceByDom(document.getElementById('memoryCharts'))?.resize();
        echarts.getInstanceByDom(document.getElementById('diskCharts'))?.resize();
    }
}

/**
 * 初始化键盘快捷键
 */
function initKeyboardShortcuts() {
    $(document).on('keydown', function(e) {
        // Ctrl + / 显示快捷键帮助
        if (e.ctrlKey && e.key === '/') {
            e.preventDefault();
            showShortcutHelp();
        }
        
        // Ctrl + M 切换侧边栏
        if (e.ctrlKey && e.key === 'm') {
            e.preventDefault();
            $('#menu_toggle').click();
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
    const shortcuts = [
        { key: 'Ctrl + M', desc: '切换侧边栏' },
        { key: 'Ctrl + /', desc: '显示快捷键帮助' },
        { key: 'ESC', desc: '关闭弹窗' }
    ];
    
    let helpContent = '<div class="shortcut-help"><h4>键盘快捷键</h4><ul>';
    shortcuts.forEach(shortcut => {
        helpContent += `<li><kbd>${shortcut.key}</kbd> - ${shortcut.desc}</li>`;
    });
    helpContent += '</ul></div>';
    
    showNotification({
        type: 'info',
        message: helpContent,
        icon: 'fa-keyboard-o'
    });
}

/**
 * 预加载资源
 */
function preloadResources() {
    const resources = [
        '/static/images/loading.gif',
        '/static/images/user.png'
    ];
    
    resources.forEach(src => {
        const img = new Image();
        img.src = src;
    });
}

/**
 * 初始化动画观察器
 */
function initAnimationObserver() {
    if ('IntersectionObserver' in window) {
        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    entry.target.classList.add('animate-in');
                }
            });
        }, { threshold: 0.1 });
        
        document.querySelectorAll('.x_panel').forEach(panel => {
            observer.observe(panel);
        });
    }
}

// 添加 CSS 动画样式
const animationStyles = `
<style>
@keyframes ripple-animation {
    to {
        transform: scale(4);
        opacity: 0;
    }
}

.modern-notification {
    position: fixed;
    top: 20px;
    right: 20px;
    background: var(--card-bg);
    border: 1px solid var(--border-color);
    border-radius: var(--border-radius);
    padding: 15px 20px;
    box-shadow: var(--shadow-xl);
    transform: translateX(100%);
    transition: all 0.3s ease;
    z-index: 10000;
    max-width: 300px;
    backdrop-filter: blur(10px);
}

.modern-notification.show {
    transform: translateX(0);
}

.modern-notification.info { border-left: 4px solid #4facfe; }
.modern-notification.warning { border-left: 4px solid #fee140; }
.modern-notification.success { border-left: 4px solid #43e97b; }

.modern-notification .close-btn {
    background: none;
    border: none;
    color: var(--text-muted);
    float: right;
    font-size: 18px;
    cursor: pointer;
    margin-left: 10px;
}

.shortcut-help {
    color: var(--text-primary);
}

.shortcut-help kbd {
    background: var(--hover-bg);
    color: var(--text-primary);
    border: 1px solid var(--border-color);
    border-radius: 4px;
    padding: 2px 6px;
    font-size: 12px;
}

.animate-in {
    animation: fadeInUp 0.6s ease forwards;
}

@media (max-width: 768px) {
    .modern-notification {
        right: 10px;
        left: 10px;
        max-width: none;
    }
}
</style>
`;

$('head').append(animationStyles);