/* 增强的侧边栏功能 - 修复折叠和子菜单问题 */

$(document).ready(function() {
    console.log('侧边栏增强功能已加载');
    
    // 清除所有可能的事件绑定
    $('#menu_toggle').off();
    $('.side-menu > li > a').off();
    
    // 菜单切换功能
    $('#menu_toggle').on('click', function(e) {
        e.preventDefault();
        console.log('菜单按钮被点击');
        
        var body = $('body');
        if (body.hasClass('nav-sm')) {
            body.removeClass('nav-sm');
            console.log('展开侧边栏');
            
            // 展开后恢复子菜单状态
            setTimeout(function() {
                $('.side-menu > li.active .child_menu').css({
                    'max-height': '500px',
                    'opacity': '1'
                });
            }, 300);
        } else {
            body.addClass('nav-sm');
            console.log('收起侧边栏');
            
            // 收起时隐藏所有子菜单
            $('.child_menu').css({
                'max-height': '0',
                'opacity': '0'
            });
        }
        
        // 移动端处理
        if ($(window).width() <= 768) {
            $('.left_col').toggleClass('mobile-show');
        }
    });
    
    // 子菜单切换功能 - 增强版
    $('.side-menu > li > a').on('click', function(e) {
        var $this = $(this);
        var $parent = $this.parent();
        var $submenu = $this.next('.child_menu');
        
        // 如果有子菜单且不是在折叠状态
        if ($submenu.length > 0 && !$('body').hasClass('nav-sm')) {
            e.preventDefault();
            console.log('子菜单被点击');
            
            // 关闭其他所有子菜单
            $('.side-menu > li').not($parent).removeClass('active');
            $('.side-menu > li').not($parent).find('.child_menu').css({
                'max-height': '0',
                'opacity': '0'
            });
            $('.side-menu > li').not($parent).find('.fa-chevron-down').removeClass('fa-chevron-up').addClass('fa-chevron-down');
            
            // 切换当前菜单
            $parent.toggleClass('active');
            
            if ($parent.hasClass('active')) {
                // 展开子菜单
                $submenu.css({
                    'max-height': '500px',
                    'opacity': '1'
                });
                // 旋转箭头
                $this.find('.fa-chevron-down').removeClass('fa-chevron-down').addClass('fa-chevron-up');
            } else {
                // 收起子菜单
                $submenu.css({
                    'max-height': '0',
                    'opacity': '0'
                });
                // 恢复箭头
                $this.find('.fa-chevron-up').removeClass('fa-chevron-up').addClass('fa-chevron-down');
            }
        }
    });
    
    // 子菜单项点击处理
    $('.child_menu li a').on('click', function(e) {
        // 移除所有子菜单项的激活状态
        $('.child_menu li a').removeClass('active');
        // 添加当前项的激活状态
        $(this).addClass('active');
    });
    
    // 移动端点击外部关闭菜单
    $(document).on('click', function(e) {
        if ($(window).width() <= 768) {
            if (!$(e.target).closest('.left_col, #menu_toggle').length) {
                $('.left_col').removeClass('mobile-show');
            }
        }
    });
    
    // 窗口大小改变处理
    $(window).on('resize', function() {
        var width = $(window).width();
        if (width > 768) {
            $('.left_col').removeClass('mobile-show');
        }
        
        // 如果在折叠状态，确保子菜单隐藏
        if ($('body').hasClass('nav-sm')) {
            $('.child_menu').css({
                'max-height': '0',
                'opacity': '0'
            });
        }
    });
    
    // 初始化：设置当前页面的菜单状态
    function initializeMenuState() {
        var currentPath = window.location.pathname;
        
        // 遍历所有子菜单链接
        $('.child_menu li a').each(function() {
            var href = $(this).attr('href');
            if (href && currentPath.includes(href) && href !== '/') {
                // 找到匹配的菜单项
                var $childMenuItem = $(this);
                var $parentMenuItem = $childMenuItem.closest('.side-menu > li');
                
                // 激活父菜单
                $parentMenuItem.addClass('active');
                
                // 展开子菜单
                var $submenu = $parentMenuItem.find('.child_menu');
                $submenu.css({
                    'max-height': '500px',
                    'opacity': '1'
                });
                
                // 旋转箭头
                $parentMenuItem.find('.fa-chevron-down').removeClass('fa-chevron-down').addClass('fa-chevron-up');
                
                // 激活子菜单项
                $childMenuItem.addClass('active');
                
                return false; // 找到后退出循环
            }
        });
    }
    
    // 页面加载完成后初始化菜单状态
    setTimeout(initializeMenuState, 100);
    
    // 添加悬停效果（仅在折叠状态下）
    $('.side-menu > li > a').on('mouseenter', function() {
        if ($('body').hasClass('nav-sm')) {
            var $this = $(this);
            var $submenu = $this.next('.child_menu');
            
            if ($submenu.length > 0) {
                // 创建悬浮提示
                var tooltip = $('<div class="nav-tooltip"></div>');
                tooltip.html($this.clone().find('span:not(.fa)').text());
                tooltip.css({
                    'position': 'fixed',
                    'left': '75px',
                    'top': $this.offset().top,
                    'background': 'var(--card-bg)',
                    'color': 'var(--text-primary)',
                    'padding': '8px 12px',
                    'border-radius': '6px',
                    'border': '1px solid var(--border-color)',
                    'box-shadow': 'var(--shadow-lg)',
                    'z-index': '10000',
                    'white-space': 'nowrap',
                    'font-size': '14px'
                });
                
                $('body').append(tooltip);
                
                // 添加子菜单内容
                if ($submenu.length > 0) {
                    var submenuItems = $submenu.find('a').map(function() {
                        return '<div style="padding: 4px 0; border-top: 1px solid var(--border-color); margin-top: 4px; padding-top: 8px;">' + 
                               '<i class="' + $(this).find('i').attr('class') + '"></i> ' + 
                               $(this).text().trim() + '</div>';
                    }).get().join('');
                    
                    if (submenuItems) {
                        tooltip.append(submenuItems);
                    }
                }
            }
        }
    }).on('mouseleave', function() {
        $('.nav-tooltip').remove();
    });
    
    console.log('侧边栏增强功能初始化完成');
});